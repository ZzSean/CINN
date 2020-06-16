#include "cinn/runtime/cinn_runtime.h"

#include <stdarg.h>
#include <stdio.h>

#include <cmath>

extern "C" {

int cinn_buffer_malloc(void* context, struct cinn_buffer_t* buf) {
  // ASSERT_NOT_NULL(context)
  ASSERT_NOT_NULL(buf)
  ASSERT_NOT_NULL(buf->device_interface)
  return buf->device_interface->impl->malloc(context, buf);
}

int cinn_buffer_free(void* context, struct cinn_buffer_t* buf) {
  // ASSERT_NOT_NULL(context)
  ASSERT_NOT_NULL(buf)
  // If buffer is lazy, then we will not free this buffer, that will greatly improve performance.
  if (buf->lazy) return 0;
  return buf->device_interface->impl->free(context, buf);
}

void* cinn_buffer_slice(struct cinn_buffer_t* buf, uint32_t offset) {
  CINN_CHECK(buf);
  uint64_t offset_byte = offset * buf->type.bytes();
  CINN_CHECK_LT(offset_byte, buf->memory_size);
  return buf->host_memory + offset_byte;
}

int cinn_device_sync(void* context, struct cinn_buffer_t* buf) {
  ASSERT_NOT_NULL(buf)
  ASSERT_NOT_NULL(buf->device_interface)
  // ASSERT_NOT_NULL(context)
  buf->device_interface->impl->sync(context, buf);
  return 0;
}

int cinn_device_release(void* context, const struct cinn_device_interface_t* device_interface) {
  // ASSERT_NOT_NULL(context)
  ASSERT_NOT_NULL(device_interface)
  CINN_NOT_IMPLEMENTED
}

int cinn_buffer_copy_to_host(void* context, struct cinn_buffer_t* buf) {
  // ASSERT_NOT_NULL(context)
  ASSERT_NOT_NULL(buf)
  ASSERT_NOT_NULL(buf->device_interface)
  return buf->device_interface->impl->copy_to_host(context, buf);
}

int cinn_buffer_copy_to_device(void* context, struct cinn_buffer_t* buf) {
  // ASSERT_NOT_NULL(context)
  ASSERT_NOT_NULL(buf)
  ASSERT_NOT_NULL(buf->device_interface)
  return buf->device_interface->impl->copy_to_device(context, buf);
}
int cinn_buffer_copy(void* context, struct cinn_buffer_t* src, struct cinn_buffer_t* dst) {
  // ASSERT_NOT_NULL(context);
  ASSERT_NOT_NULL(src);
  ASSERT_NOT_NULL(dst);
  return dst->device_interface->buffer_copy(context, src, dst);
}

void* cinn_buffer_get_data_handle(struct cinn_buffer_t* buf) {
  CINN_CHECKP(buf, "%s", "buffer is null");
  return buf->host_memory;
}

void* cinn_buffer_get_data_const_handle(const struct cinn_buffer_t* buf) {
  CINN_CHECKP(buf, "%s", "buffer is null");
  return buf->host_memory;
}

cinn_type_t cinn_unk_t() { return cinn_type_t(cinn_type_unk, 0); }
cinn_type_t cinn_int32_t() { return cinn_type_t(cinn_type_int, 32); }
cinn_type_t cinn_int64_t() { return cinn_type_t(cinn_type_int, 64); }
cinn_type_t cinn_uint32_t() { return cinn_type_t(cinn_type_uint, 32); }
cinn_type_t cinn_uint64_t() { return cinn_type_t(cinn_type_uint, 64); }
cinn_type_t cinn_float32_t() { return cinn_type_t(cinn_type_float, 32); }
cinn_type_t cinn_float64_t() { return cinn_type_t(cinn_type_float, 64); }

}  // extern "C"

struct cinn_buffer_t* cinn_buffer_t::new_(cinn_device_kind_t device,
                                          cinn_type_t type,
                                          const std::vector<int>& shape,
                                          int align) {
  int32_t dimensions     = shape.size();
  cinn_dimension_t* dims = (cinn_dimension_t*)malloc(sizeof(cinn_dimension_t) * dimensions);  // NOLINT
  memcpy(dims, shape.data(), shape.size() * sizeof(int));

  struct cinn_buffer_t* x = (struct cinn_buffer_t*)malloc(sizeof(struct cinn_buffer_t));
  x->type                 = type;
  x->device               = device;
  x->host_memory          = nullptr;
  x->memory_size          = 0;
  x->lazy                 = true;
  // NOTE set device_interface for each buffer.
  switch (x->device) {
    case cinn_x86_device:
      x->device_interface = cinn_x86_device_interface();
      break;
    case cinn_unk_device:
      fprintf(stderr, "Device type of buffer should be set, found Unk");
      abort();
      break;
    default:
      fprintf(stderr, "Not supported device type");
      abort();
  }

  x->dims       = dims;
  x->dimensions = dimensions;
  x->align      = align;
  return x;
}

cinn_buffer_t* cinn_buffer_new(cinn_device_kind_t device, cinn_type_t type, const std::vector<int>& shape, int align) {
  return cinn_buffer_t::new_(device, type, shape, align);
}

//! Implement the type_code for all the supported types.
// @{
#define __m(T, code__)                             \
  template <>                                      \
  constexpr int cinn_pod_value_t::type_code<T>() { \
    return code__;                                 \
  }
__m(int32_t, 0);
__m(int64_t, 1);
__m(float, 2);
__m(double, 3);
__m(void*, 4);
__m(char*, 5);
__m(char const*, 6);
__m(cinn_buffer_t*, 7);
#undef __m
//@}

cinn_pod_value_t::operator double() const {
  CINN_CHECK_EQ(type_code_, type_code<double>());
  return value_.v_float64;
}
cinn_pod_value_t::operator float() const {
  CINN_CHECK_EQ(type_code_, type_code<float>());
  return value_.v_float64;
}
cinn_pod_value_t::operator int32_t() const {
  CINN_CHECK_EQ(type_code_, type_code<int32_t>());
  return value_.v_int64;
}
cinn_pod_value_t::operator int64_t() const {
  CINN_CHECK_EQ(type_code_, type_code<int64_t>());
  return value_.v_int64;
}
cinn_pod_value_t::operator void*() const {
  CINN_CHECK_EQ(type_code_, type_code<void*>());
  return value_.v_handle;
}
cinn_pod_value_t::operator cinn_buffer_t*() const {
  CINN_CHECK_EQ(type_code_, type_code<cinn_buffer_t*>());
  return static_cast<cinn_buffer_t*>(value_.v_handle);
}
cinn_pod_value_t::operator char*() const {
  CINN_CHECK_EQ(type_code_, type_code<char*>());
  return static_cast<char*>(value_.v_handle);
}

cinn_pod_value_t::cinn_pod_value_t(cinn_value_t value, int type_code) : value_(value), type_code_(type_code) {}
cinn_pod_value_t::cinn_pod_value_t(cinn_buffer_t* value) : type_code_(type_code<cinn_buffer_t*>()) {
  value_.v_handle = value;
}
cinn_pod_value_t::cinn_pod_value_t(int32_t value) : type_code_(type_code<int32_t>()) { value_.v_int64 = value; }
cinn_pod_value_t::cinn_pod_value_t(int64_t value) : type_code_(type_code<int64_t>()) { value_.v_int64 = value; }
cinn_pod_value_t::cinn_pod_value_t(float value) : type_code_(type_code<float>()) { value_.v_float64 = value; }
cinn_pod_value_t::cinn_pod_value_t(double value) : type_code_(type_code<double>()) { value_.v_float64 = value; }
cinn_pod_value_t::cinn_pod_value_t(void* value) : type_code_(type_code<void*>()) { value_.v_handle = value; }
cinn_pod_value_t::cinn_pod_value_t(const char* value) : type_code_(type_code<char*>()) {
  value_.v_handle = const_cast<char*>(value);
}

// @{
float cinn_pod_value_to_float(cinn_pod_value_t* value) { return *value; }
double cinn_pod_value_to_double(cinn_pod_value_t* value) { return *value; }
int64_t cinn_pod_value_to_int64(cinn_pod_value_t* value) { return *value; }
int32_t cinn_pod_value_to_int32(cinn_pod_value_t* value) { return *value; }
void* cinn_pod_value_to_void_p(cinn_pod_value_t* value) { return *value; }
cinn_buffer_t* cinn_pod_value_to_buffer_p(cinn_pod_value_t* value) { return *value; }
// @}

// @{
void float_to_cinn_pod_value(float v, cinn_pod_value_t* out) { *out = cinn_pod_value_t(v); }
void int32_to_cinn_pod_value(int32_t v, cinn_pod_value_t* out) { *out = cinn_pod_value_t(v); }

void handle_to_cinn_pod_value(void* v, cinn_pod_value_t* out) { *out = cinn_pod_value_t(v); }
void buffer_p_to_cinn_pod_value(const cinn_buffer_t* v, cinn_pod_value_t* out) {
  *out = cinn_pod_value_t(const_cast<cinn_buffer_t*>(v));
}
// @}

void cinn_print_debug_string(const char* s, ...) {
  va_list args;
  va_start(args, s);
  vfprintf(stderr, s, args);
  va_end(args);

  fprintf(stderr, "\n");
}

void debug_pod_value(cinn_pod_value_t v, int i) {
  switch (v.type_code()) {
    case cinn_pod_value_t::type_code<cinn_buffer_t*>(): {
      cinn_buffer_t* node = v;
      if (node->host_memory) {
        cinn_print_debug_string("arg[%d].host_memory: %p\n", i, node->host_memory);
      } else {
        cinn_print_debug_string("arg[%d].host_memory: %p\n", i, NULL);
      }
    } break;
    case cinn_pod_value_t::type_code<int32_t>(): {
      int node = v;
      cinn_print_debug_string("arg[%d] : %d\n", i, node);
    } break;
    case cinn_pod_value_t::type_code<float>(): {
      float node = v;
      cinn_print_debug_string("arg[%f] : %d\n", i, node);
    } break;
    default:
      cinn_print_debug_string("pod type not supported");
      break;
  }
}

void cinn_print_debug_args(cinn_pod_value_t* args, int count) {
  cinn_print_debug_string("start debug ==");
  cinn_print_debug_string("args: %p\n", (void*)args);  // NOLINT
  cinn_print_debug_string("with %d arguments", count);
  if (!args) {
    cinn_print_debug_string("args is null!!");
    return;
  }

  for (int i = 0; i < count; i++) {
    cinn_print_debug_string("arg[%d]: %p\n", i, (void*)(&args[i]));  // NOLINT
    debug_pod_value(args[i], i);
  }
}

void cinn_args_construct(cinn_pod_value_t* arr, int count, ...) {
  CINN_CHECK(count < 1000);

  va_list args;
  va_start(args, count);
  for (int i = 0; i < count; i++) {
    cinn_pod_value_t* elem_addr = va_arg(args, cinn_pod_value_t*);
    arr[i]                      = *elem_addr;
    // debug_pod_value(*elem_addr, i);
  }
  va_end(args);
}

void* cinn_pod_value_t::data_addr() const {
  switch (type_code()) {
    case cinn_pod_value_t::type_code<int32_t>():
    case cinn_pod_value_t::type_code<int64_t>():
      return (void*)&value_.v_int64;  // NOLINT
    case cinn_pod_value_t::type_code<float>():
      return (void*)&value_.v_float64;  // NOLINT
    case cinn_pod_value_t::type_code<void*>():
      return (void*)&value_.v_handle;  // NOLINT
    case cinn_pod_value_t::type_code<cinn_buffer_t*>():
      return (void*)&value_.v_handle;  // NOLINT
    default:
      cinn_print_debug_string("POD value type [%d] not supported", type_code());
      CINN_NOT_IMPLEMENTED
  }
  return nullptr;
}

#include "cinn/runtime/cinn_x86_device_impl.cc"
